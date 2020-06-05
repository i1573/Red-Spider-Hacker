// KillerGUI.cpp : 定义主程序的入口点。

// 版权声明：
// Copyright (C) 2013-2019 ETO.                   All Rights Reserved.
// Copyright (C) 2018-2019 FreeSTD Inc.           All Rights Reserved.
// Copyright (C) 2018-2019 Chengdu Zuosi Co.Ltd. All Rights Reserved.

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define PROGRAM_DISPLAY_NAME L"红蜘蛛多媒体网络教室学生端破解程序1.0 beta 1"
#define PROGRAM_DISPLAY_NAME_A "红蜘蛛多媒体网络教室学生端破解程序1.0 beta 1"

#include "framework.h"
#include "KillerGUI.h"
#include <Windows.h>
#include <TlHelp32.h>
#include <comdef.h>
#include <shlobj.h>
#include <atlconv.h>
#include <CommCtrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <thread>
#include <string>
#include <sstream>
#include <fstream>

#pragma comment(lib , "Comctl32.lib")
#define KEY_DOWN(VK_NONAME) ((GetAsyncKeyState(VK_NONAME) & 0x8000) ? 1 : 0)
#define WM_NOTIFYICON WM_USER + 0x01

using std::thread;
using std::string;
using std::stringstream;
using std::ifstream;
using std::ofstream;

// 全局变量
// 主程序
HINSTANCE hInst;                                // 当前实例
HWND hWnd;                                      // 主窗口句柄
HWND TrayhWnd;                                  // 托盘图标窗口句柄
NOTIFYICONDATA nid = { 0 };                     // 托盘图标数据
double DPI;                                     // 高DPI显示比例
bool isWindow;                                  // 是否为窗口模式

// 禁用黑屏肃静教师演示
bool isTeacherScreenKillerEnabled;              // 禁用黑屏肃静教师演示是否打开
bool isTeacherScreenShow;                       // 黑屏肃静教师演示是否显示
int isTeacherScreenBox;                         // 黑屏肃静教师演示是否再次提示

// 此代码模块中包含的函数的前向声明
void             CreateTray(HINSTANCE hInstance);
LRESULT CALLBACK TrayProc(HWND, UINT, WPARAM, LPARAM);
void             CreateMainWindow(HINSTANCE hInstance);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void             APIHookChecker();
void             APIHookInstaller();
void             TeacherScreenKiller();
void             TeacherScreenKillerBox(HWND RedHWND);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

	// 初始化工作目录
	char strModule[256];
	GetModuleFileNameA(NULL, strModule, 256);
	std::string a;
	a.assign(strModule);
	a.append("\\..\\");
	SetCurrentDirectoryA(a.c_str());

	// 载入设置
	isWindow = 1;
	isTeacherScreenKillerEnabled = 0;
	isTeacherScreenBox = 1;

	// 获取高DPI显示比例
	int _TDPI = GetDeviceCaps(GetDC(NULL), LOGPIXELSX);
	DPI = _TDPI / 96.0;

	// 创建托盘图标
	CreateTray(hInstance);
    // 创建主窗口
	CreateMainWindow(hInstance);

	// 提前启动功能
	thread AHC(APIHookChecker);
	AHC.detach();
	thread TSK(TeacherScreenKiller);
	TSK.detach();
	isTeacherScreenKillerEnabled = 1;

	// 主消息循环
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_KILLERGUI));
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}


void CreateTray(HINSTANCE hInstance)
{
	// 注册窗口类
	WNDCLASSEXW wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = TrayProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_KILLERGUI));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_KILLERGUI);
	wcex.lpszClassName = L"KILLERGUI_V2_TRAY";
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	RegisterClassExW(&wcex);

	// 将实例句柄存储在全局变量中
	hInst = hInstance;

	TrayhWnd = CreateWindowW(L"KILLERGUI_V2_TRAY", L"TrayWindow", WS_POPUPWINDOW,
		0, 0, 0, 0, nullptr, nullptr, hInstance, nullptr);

	// 检查是否创建成功
	if (!TrayhWnd)
	{
		MessageBox(NULL, L"创建托盘图标窗口失败！", PROGRAM_DISPLAY_NAME, MB_OK | MB_ICONERROR);
		::exit(0);
	}

	// 刷新窗口
	ShowWindow(TrayhWnd, SW_HIDE);
	UpdateWindow(TrayhWnd);

	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = TrayhWnd;
	nid.uID = IDI_SMALL;
	nid.hIcon = ::LoadIcon(hInst, MAKEINTRESOURCE(IDI_SMALL));
	nid.uCallbackMessage = WM_NOTIFYICON;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	_tcscpy_s(nid.szTip, PROGRAM_DISPLAY_NAME);
	Shell_NotifyIcon(NIM_ADD, &nid);
}
LRESULT CALLBACK TrayProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// 分析菜单选择:
		switch (wmId)
		{
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_NOTIFYICON:
		if ((wParam == IDI_SMALL) && (lParam == WM_LBUTTONDOWN))
		{
			ShowWindow(::hWnd, SW_SHOW);
			SetForegroundWindow(::hWnd);
		}
		else if ((wParam == IDI_SMALL) && (lParam == WM_RBUTTONDOWN))
		{
			POINT pt;
			GetCursorPos(&pt);
			SetForegroundWindow(::hWnd);
			HMENU hMenu, hMenu1;
			hMenu = LoadMenu(hInst, MAKEINTRESOURCEW(IDC_NOTIFYICON));
			hMenu1 = GetSubMenu(hMenu, 0);
			WPARAM ReturnMsg;
			ReturnMsg = TrackPopupMenu(hMenu1, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, pt.x, pt.y, NULL, hWnd, NULL);
			if (ReturnMsg > 0)
			{
				SendMessage(::hWnd, WM_COMMAND, ReturnMsg, 0);
				ShowWindow(::hWnd, SW_SHOW);
				SetForegroundWindow(::hWnd);
			}
		}
		break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: 在此处添加使用 hdc 的任何绘图代码...
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void CreateMainWindow(HINSTANCE hInstance)
{
	// 注册窗口类
	WNDCLASSEXW wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_KILLERGUI));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = CreatePatternBrush((HBITMAP)LoadImage(NULL, L"Background.bmp", IMAGE_BITMAP, 950 * DPI, 425 * DPI, LR_LOADFROMFILE));
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_KILLERGUI);
	wcex.lpszClassName = L"KILLERGUI_V2";
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	RegisterClassExW(&wcex);

	// 将实例句柄存储在全局变量中
	hInst = hInstance;

	// 检查分辨率
	int scrWidth, scrHeight;
	scrWidth = (GetSystemMetrics(SM_CXSCREEN) - 960 * DPI) / 2;
	scrHeight = (GetSystemMetrics(SM_CYSCREEN) - 480 * DPI) / 2;
	if (scrWidth < 0 || scrHeight < 0)
	{
		USES_CONVERSION;
		int DPIPercent = DPI * 100;
		string sDPIPercent, sW, sH;
		stringstream tempIO, tempIO1, tempIO2;
		tempIO << DPIPercent;
		tempIO >> sDPIPercent;
		tempIO1 << 960 * DPI;
		tempIO1 >> sW;
		tempIO2 << 480 * DPI;
		tempIO2 >> sH;
		string Message = (string)""
			+ "对于显示比例设置为 " + sDPIPercent + "% 的计算机，本程序需要 " + sW + " × " + sH + " 或更高分辨率以达到最优效果。\n"
			+ "要解决此问题，可以尝试以下方法之一：\n"
			+ "  ● 增加分辨率\n"
			+ "  ● 减小显示比例\n";
		InitCommonControls();
		int nButtonPressed = 0;
		TASKDIALOGCONFIG config = { 0 };
		const TASKDIALOG_BUTTON buttons[] = {
			{ IDOK, L"忽略该问题，继续运行程序\n程序的某些模块可能会显示错误" },
			{ 100, L"退出程序\n稍后您可以重新打开程序" }
		};
		config.cbSize = sizeof(config);
		config.hInstance = hInst;
		config.dwFlags = TDF_USE_COMMAND_LINKS;
		config.dwCommonButtons = 0;
		config.pszMainIcon = TD_WARNING_ICON;
		config.pszWindowTitle = PROGRAM_DISPLAY_NAME;
		config.pszMainInstruction = L"检查您的显示设置";
		config.pszContent = A2W(Message.c_str());
		config.pButtons = buttons;
		config.cButtons = ARRAYSIZE(buttons);
		TaskDialogIndirect(&config, &nButtonPressed, NULL, NULL);
		switch (nButtonPressed)
		{
		case IDOK:
			break;
		case 100:
			::exit(0);
		default:
			break;
		}
	}

	// 创建主窗口
	if (isWindow)
		hWnd = CreateWindowW(L"KILLERGUI_V2", L"", WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX,
			scrWidth, scrHeight, 960 * DPI, 480 * DPI, nullptr, nullptr, hInstance, nullptr);
	else
		hWnd = CreateWindowW(L"KILLERGUI_V2", L"", WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX,
			scrWidth, scrHeight, 960 * DPI, 56 * DPI, nullptr, nullptr, hInstance, nullptr);
	
	// 检查是否创建成功
	if (!hWnd)
	{
		MessageBox(NULL, L"创建主窗口失败！", PROGRAM_DISPLAY_NAME, MB_OK | MB_ICONERROR);
		::exit(0);
	}

	// 刷新窗口
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);
		nid.uFlags = NIF_INFO | NIF_GUID;
		nid.dwInfoFlags = 0x00000004;
		_tcscpy_s(nid.szInfo, L"程序将在后台继续运行，单击托盘恢复窗口。");
		_tcscpy_s(nid.szTip, PROGRAM_DISPLAY_NAME);
		lstrcpy(nid.szInfoTitle, PROGRAM_DISPLAY_NAME);
		nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_SMALL));
		Shell_NotifyIcon(NIM_MODIFY, &nid);
		break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择:
            switch (wmId)
            {
			case ID_Window:
			{
				int tempY;
				tempY = (isWindow ? (480 - 4) * DPI : (56 + 4) * DPI);
				for (int i = 1; i <= 35; i++)
				{
					tempY += (isWindow ? -12 * DPI : 12 * DPI);
					SetWindowPos(hWnd, NULL, NULL, NULL, 960 * DPI, tempY, SWP_NOMOVE | SWP_NOZORDER);
				}
				isWindow = !isWindow;
			}
			break;
			case ID_TSKEn:
				if (isTeacherScreenKillerEnabled)
					MessageBox(hWnd, L"该功能已启用！", PROGRAM_DISPLAY_NAME, MB_OK | MB_ICONERROR);
				else
				{
					thread TSK(TeacherScreenKiller);
					TSK.detach();
					isTeacherScreenKillerEnabled = 1;
					MessageBox(hWnd, L"成功禁用了黑屏肃静教师演示！", PROGRAM_DISPLAY_NAME, MB_OK | MB_ICONINFORMATION);
				}
				break;
			case ID_TSKDis:
				MessageBox(hWnd, L"该功能有待开发！", PROGRAM_DISPLAY_NAME, MB_OK | MB_ICONERROR);
				break;
            case IDM_ABOUT:
				MessageBox(hWnd, L"Copyright (C) 2013-2019 ETO.\nCopyright (C) 2018-2019 FreeSTD Inc.\nCopyright (C) 2018-2019 Chengdu Zuosi Co.Ltd.\nAll Rights Reserved.", PROGRAM_DISPLAY_NAME, MB_OK | MB_ICONINFORMATION);
                break;
			case ID_SHOW:
				ShowWindow(hWnd, SW_SHOW);
				SetForegroundWindow(hWnd);
				break;
            case IDM_EXIT:
			{
				InitCommonControls();
				int nButtonPressed = 0;
				TASKDIALOGCONFIG config = { 0 };
				const TASKDIALOG_BUTTON buttons[] = {
					{ IDOK, L"继续退出\n您将不会受到保护" },
					{ 100, L"暂不退出" }
				};
				config.cbSize = sizeof(config);
				config.hInstance = hInst;
				config.hwndParent = hWnd;
				config.dwFlags = TDF_USE_COMMAND_LINKS;
				config.dwCommonButtons = 0;
				config.pszMainIcon = TD_WARNING_ICON;
				config.pszWindowTitle = L"退出";
				config.pszMainInstruction = L"退出程序";
				config.pszContent = L"确实要退出程序吗？";
				config.pButtons = buttons;
				config.pszVerificationText = L"不再显示该信息";
				config.cButtons = ARRAYSIZE(buttons);
				TaskDialogIndirect(&config, &nButtonPressed, NULL, &isTeacherScreenBox);
				isTeacherScreenBox = !isTeacherScreenBox;
				switch (nButtonPressed)
				{
				case IDOK:
					DestroyWindow(hWnd);
				case 100:
					break;
				default:
					break;
				}
			}
            break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 在此处添加使用 hdc 的任何绘图代码...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
		nid.cbSize = sizeof(NOTIFYICONDATA);
		nid.hWnd = TrayhWnd;
		nid.uID = IDI_SMALL;
		Shell_NotifyIcon(NIM_DELETE, &nid);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int CheckProcess(std::string s)
{
	HANDLE hToken;
	OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken);
	TOKEN_PRIVILEGES tp;
	LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid);
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
	PROCESSENTRY32 pd;
	pd.dwSize = sizeof(pd);
	HANDLE hProcessSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	BOOL choose = ::Process32First(hProcessSnap, &pd);
	int stdexe = 0;
	while (choose)
	{
		char szEXE[100];
		_bstr_t b(pd.szExeFile);
		strcpy_s(szEXE, b);
		if (!strcmp(szEXE, s.c_str()))
			++stdexe;
		choose = ::Process32Next(hProcessSnap, &pd);
	}
	return stdexe;
}
void StopProcess(std::string s)
{
	HANDLE hToken;
	OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken);
	TOKEN_PRIVILEGES tp;
	LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid);
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
	PROCESSENTRY32 pd;
	pd.dwSize = sizeof(pd);
	HANDLE hProcessSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	BOOL choose = ::Process32First(hProcessSnap, &pd);
	while (choose)
	{
		char szEXE[100];
		_bstr_t b(pd.szExeFile);
		strcpy_s(szEXE, b);
		if (!strcmp(szEXE, s.c_str()))
		{
			HANDLE std = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pd.th32ProcessID);
			::TerminateProcess(std, 0);
		}
		choose = ::Process32Next(hProcessSnap, &pd);
	}
}

void APIHookChecker()
{
	bool isHook = 0;
	while (1)
	{
		if (isHook != CheckProcess("REDAgent.exe"))
			if (isHook)
				isHook = 0;
			else
			{
				isHook = 1;
				Sleep(500);
				APIHookInstaller();
			}
		Sleep(500);
	}
}
void APIHookInstaller()
{
	std::string UserTempPath;
	char ttp[MAX_PATH];
	GetTempPathA(260, ttp);
	UserTempPath = ttp;
	const DWORD dwThreadSize = 5 * 1024;
	DWORD dwWriteBytes, dwProcessId;
	HANDLE hToken;
	OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken);
	TOKEN_PRIVILEGES tp;
	LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid);
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
	PROCESSENTRY32 pd;
	pd.dwSize = sizeof(pd);
	HANDLE hProcessSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	BOOL choose = ::Process32First(hProcessSnap, &pd);
	while (choose)
	{
		char szEXE[100];
		_bstr_t b(pd.szExeFile);
		strcpy_s(szEXE, b);
		if (!strcmp(szEXE, "REDAgent.exe"))
		{
			dwProcessId = pd.th32ProcessID;
			break;
		}
		choose = ::Process32Next(hProcessSnap, &pd);
	}
	if (dwProcessId == 0)
	{
		MessageBox(hWnd, L"错误：无法注入DLL：APIHook.dll，未找到进程REDAgent.exe!", PROGRAM_DISPLAY_NAME, MB_ICONERROR | MB_OK);
		return;
	}


	HANDLE hTargetProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
	if (!hTargetProcess)
	{
		MessageBox(hWnd, L"错误：无法注入DLL：APIHook.dll，无法操作进程REDAgent.exe!", PROGRAM_DISPLAY_NAME, MB_ICONERROR | MB_OK);
		return;
	}


	void* pRemoteThread = VirtualAllocEx(hTargetProcess,
		0,
		dwThreadSize,
		MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (!pRemoteThread)
		return;

	char szDll[5120];
	memset(szDll, 0, 5120);
	CopyFileA("APIHook.dll", (UserTempPath + "APIHook.dll").c_str(), FALSE);
	strcpy_s(szDll, (UserTempPath + "APIHook.dll").c_str());

	if (!WriteProcessMemory(hTargetProcess,
		pRemoteThread,
		(LPVOID)szDll,
		dwThreadSize,
		0))
	{
		MessageBox(hWnd, L"错误：无法注入DLL：APIHook.dll，无法执行WriteProcessMemory！", PROGRAM_DISPLAY_NAME, MB_ICONERROR | MB_OK);
		return;
	}

	LPVOID pFunc = LoadLibraryA;

	HANDLE hRemoteThread = CreateRemoteThread(hTargetProcess,
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)pFunc,
		pRemoteThread,
		0,
		&dwWriteBytes);
	if (!hRemoteThread)
	{
		MessageBox(hWnd, L"错误：无法注入DLL：APIHook.dll，无法创建线程!", PROGRAM_DISPLAY_NAME, MB_ICONERROR | MB_OK);
		return;
	}


	WaitForSingleObject(hRemoteThread, INFINITE);
	VirtualFreeEx(hTargetProcess, pRemoteThread, dwThreadSize, MEM_COMMIT);
	CloseHandle(hRemoteThread);
}
void TeacherScreenKiller()
{
	isTeacherScreenKillerEnabled = 1;
	isTeacherScreenShow = 0;
	HWND RedHWND = NULL;
	while (1)
	{
		while (!RedHWND)
		{
			RedHWND = ::FindWindow(L"DIBFullViewClass", NULL);
			Sleep(100);
		}
		if (isTeacherScreenBox)
		{
			thread TSBox(TeacherScreenKillerBox, RedHWND);
			TSBox.detach();
		}
		isTeacherScreenShow = 1;
		while (RedHWND)
		{
			if (KEY_DOWN(VK_CONTROL) && KEY_DOWN(VK_SPACE))
			{
				if (isTeacherScreenShow)
					ShowWindow(RedHWND, SW_HIDE);
				else
					ShowWindow(RedHWND, SW_SHOW);
				isTeacherScreenShow = !isTeacherScreenShow;
				Sleep(400);
			}
			RedHWND = ::FindWindow(L"DIBFullViewClass", NULL);
			Sleep(100);
		}
		HWND temp = ::FindWindow(NULL, L"禁用黑屏肃静教师演示");
		if (temp)
			SendMessage(temp, TDM_CLICK_BUTTON, 100, 0);
		isTeacherScreenShow = 0;
	}
}
void TeacherScreenKillerBox(HWND RedHWND)
{
	InitCommonControls();
	int nButtonPressed = 0;
	TASKDIALOGCONFIG config = { 0 };
	const TASKDIALOG_BUTTON buttons[] = {
		{ IDOK, L"立即隐藏红蜘蛛黑屏肃静教师演示\n您可以稍后再次按下“Ctrl + 空格”恢复" },
		{ 100, L"暂时忽略\n您可以稍后自行按下“Ctrl + 空格”隐藏" }
	};
	config.cbSize = sizeof(config);
	config.hInstance = hInst;
	config.hwndParent = RedHWND;
	config.dwFlags = TDF_USE_COMMAND_LINKS;
	config.dwCommonButtons = 0;
	config.pszMainIcon = TD_WARNING_ICON;
	config.pszWindowTitle = L"禁用黑屏肃静教师演示";
	config.pszMainInstruction = L"隐藏红蜘蛛黑屏肃静教师演示";
	config.pszContent = L"您可以现在立即隐藏，或稍后按下键盘“Ctrl + 空格”隐藏";
	config.pButtons = buttons;
	config.pszVerificationText = L"不再显示该信息";
	config.cButtons = ARRAYSIZE(buttons);
	TaskDialogIndirect(&config, &nButtonPressed, NULL, &isTeacherScreenBox);
	isTeacherScreenBox = !isTeacherScreenBox;
	switch (nButtonPressed)
	{
	case IDOK:
		ShowWindow(RedHWND, SW_HIDE);
		isTeacherScreenShow = 0;
		break;
	case 100:
		break;
	default:
		break;
	}
}