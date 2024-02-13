// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include <Windows.h>
#include <iostream>
#include <string>
#include "CasioEditor.h"
// 导出函数
class CSegment {
public:
	char Unk[4];
	::byte* Contents;
	size_t Size;
	char Unk2[10];
	int StartAddress;
	int EndAddress;
};
class CSimU8Core {
public:
	CSegment* GetCodeSegment0() {
		return (CSegment*)((char*)this + 0x2C);
	}
	CSegment* GetDataSegment0() {
		return (CSegment*)((char*)this + 0x48);
	}
	CSegment* GetDataSegment1() {
		return (CSegment*)((char*)this + 0x64);
	}
};
CSimU8Core* GetCSimU8Core() {
	auto simu8 = GetModuleHandle(L"Real_SimU8.dll");
	return (CSimU8Core*)((char*)simu8 + 0x16BE28);
}

void* GetInputArea(CSegment* seg0) {
	return seg0->Contents + 0x9268;
}

void Reset68() {
	auto core = GetCSimU8Core();
	auto seg0 = core->GetDataSegment0();
	memset(seg0->Contents + 0x9068, 0, 0x200);
}
void CopyInput() {
	auto core = GetCSimU8Core();
	auto seg0 = core->GetDataSegment0();
	auto global = GlobalAlloc(0, 200);
	auto ptr = GlobalLock(global);
	memcpy(ptr, seg0->Contents + 0x9268, 200);
	GlobalUnlock(global);
	auto res = OpenClipboard(0);
	auto res2 = EmptyClipboard();
	auto res3 = SetClipboardData(CF_TEXT,global);
	auto res4 = CloseClipboard();
}
void PasteInput() {
	auto core = GetCSimU8Core();
	auto seg0 = core->GetDataSegment0();

	auto res = OpenClipboard(0);

	auto global = GetClipboardData(CF_TEXT);
	auto ptr = GlobalLock(global);
	memcpy(seg0->Contents + 0x9268, ptr, 200);
	GlobalUnlock(global);

	auto res4 = CloseClipboard();
}
void ClearHist() {
	auto core = GetCSimU8Core();
	auto seg0 = core->GetDataSegment0();
	memset(seg0->Contents + 0x93f8, 0, 400);
}
LRESULT __stdcall WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_PRINT: {
		auto hdc = GetDC(hWnd);
		RECT r{};
		GetClientRect(hWnd, &r);
		FillRect(hdc, &r, (HBRUSH)GetStockObject(WHITE_BRUSH));
		DeleteDC(hdc);
		break;
	}
	case WM_CLOSE: {
		return 0;
	}
	case WM_COMMAND: {
		switch (LOWORD(wParam))
		{
		case 1:
			Reset68();
			break;
		case 2:
			CopyInput();
			break;
		case 3:
			PasteInput();
			break;
		case 4:
			ClearHist();
			break;
		}
	}
	default:
		break;
	}
	return DefWindowProcW(hWnd, Msg, wParam, lParam);
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex{};

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszClassName = L"CASIOHACK";

	return RegisterClassExW(&wcex);
}
HFONT textFont = 0;

void LoadFont() {
	textFont = CreateFont(
		20,                 // 字体高度
		0,                  // 字体宽度
		0,                  // 字体倾斜角度
		0,                  // 字体方向角度
		FW_NORMAL,          // 字体粗细程度
		FALSE,              // 是否为斜体
		FALSE,              // 是否有下划线
		FALSE,              // 是否有删除线
		DEFAULT_CHARSET,    // 字符集
		OUT_DEFAULT_PRECIS, // 输出精度
		CLIP_DEFAULT_PRECIS,// 剪辑精度
		DEFAULT_QUALITY,    // 字体质量
		DEFAULT_PITCH | FF_DONTCARE,    // 字体族和字体名称
		L"Microsoft YaHei UI"            // 字体名称
	);
}
void Btn(const wchar_t* text, int x, int y, int cx, int cy, HWND win, int id = 0) {
	auto btn = CreateWindow(L"BUTTON", text, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		x, y, cx, cy, win, (HMENU)id, 0, 0);
	SendMessage(btn, WM_SETFONT, (WPARAM)textFont, TRUE);
}

DWORD __stdcall load(LPVOID) {
	auto hinst = GetModuleHandleW(0);
	MyRegisterClass(hinst);
	auto win = CreateWindowExW(WS_EX_TOOLWINDOW,
		L"CASIOHACK", L"CWII Control Panel",
		WS_OVERLAPPEDWINDOW & ~WS_SYSMENU, 0, 0, 800, 600, 0, 0, hinst, 0);
	LoadFont();
	Btn(L"Reset 68", 720, 0, 60, 30, win, 1);
	Btn(L"Copy", 720, 40 * 1, 60, 30, win, 2);
	Btn(L"Paste", 720, 40 * 2, 60, 30, win, 3);
	Btn(L"Clr.Hist", 720, 40 * 3, 60, 30, win, 4);
	new Editor(0,0,400,400,win,5);
	ShowWindow(win, SW_SHOW);
	SendMessageW(win, WM_PRINT, 0, 0);
	return 0;
}


BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hModule);
		load(0);
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

