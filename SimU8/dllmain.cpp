// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include <Windows.h>
#include <MinHook.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <format>
#include <filesystem>
#include "SimU8_Decl.h"
#include "SimU8_Core.h"
#include "cwii_op.h"
#include "StbImage.h"
#include <bitset>

void PrintBuffer(const void* lpBuffer, DWORD dwSize)
{
	if (dwSize == 0 || lpBuffer == 0)
	{
		printf("<NULL>");
		return;
	}
	printf("             0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
	for (DWORD i = 0; i < dwSize; i++)
	{
		if ((i) % 16 == 0)
		{
			printf("0x%08X | ", i);
		}
		printf("%02X ", ((BYTE*)lpBuffer)[i]);
		if ((i + 1) % 16 == 0)
		{
			printf("\n");
		}
	}
	printf("\n");
}

struct CharInfo {
	USHORT id;
	USHORT x;
	USHORT y;
	byte width;
	byte height;
};
std::vector< CharInfo> casio_font_infos;
Image casio_font_atlas;
HDC font_dc;
HBITMAP font_bmp;
void loadCasioFont() {
	if (std::filesystem::exists("font.png")) {
		casio_font_atlas = Image{ "font.png",4 };
		auto bytes = readFileBytes("font.bin");
		auto count = *(int*)bytes.data();
		casio_font_infos.resize(count);
		memcpy(casio_font_infos.data(), ((int*)bytes.data()) + 1, count * 8);
		font_bmp = casio_font_atlas.GenerateDIBitmap();
		font_dc = CreateCompatibleDC(GetDC(0));
		SelectObject(font_dc, font_bmp);
	}
}
CharInfo lookupCasioAtlas(int id) {
	for (auto info : casio_font_infos)
	{
		if (info.id == id)
			return info;
	}
	return {};
}


void renderCasio(HDC hdc, const char* hex, int x, int y, int Width) {
	int cx = x;
	int cy = y;
	USHORT b = 0;
	for (int i = 0; hex[i] != 0; i++) {
		auto p = hex[i] & 0xff;
		if (p >= 0xF0)
		{
			b = p << 8 | (hex[i + 1] & 0xff);
			i++;
		}
		else
		{
			b = p;
		}
		auto chr = lookupCasioAtlas(b);
		if (chr.id == 0) {
			chr = lookupCasioAtlas(0x01);
		}
		if (chr.id != 0)
		{
			if (cx + chr.width > Width)
			{
				cx = x;
				cy += 14;
			}
			BitBlt(hdc, cx, cy, chr.width, chr.height - 1, font_dc, chr.x, chr.y, SRCCOPY);
			cx += chr.width + 1;
		}
	}
}

std::string toBinaryString(unsigned char number) {
	return std::bitset<8>(number).to_string();
}
void RenderInput(HDC hdc) {
	auto core = GetCSimU8Core();
	auto dataseg = core->GetDataSegment0();
	RECT r{ 0,40 * 6,680,600 };
	FillRect(hdc, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));
	renderCasio(hdc, (char*)GetInputArea(dataseg), 0, 40 * 6, 680);
	DeleteDC(hdc);
}
HFONT textFont = 0;
LRESULT __stdcall WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_PAINT: {
		break;
	}
	case WM_TIMER: {
		auto core = GetCSimU8Core();
		std::string view_str;
		for (size_t i = 0; i < 16; i++)
		{
			view_str += "R";
			auto ss = std::to_string(i);
			if (ss.size() == 1)
			{
				ss.insert(ss.begin(), '0');
			}
			view_str += ss;
			view_str += ": ";
			view_str += to_hex(core->registers[i]);
			view_str += " ";
			if ((i + 1) % 4 == 0)
			{
				view_str += "\n";
			}
		}
		core->cyclecount = 0;
		auto final_view =
			std::format(
				"{}\n"
				"PC: 0x{:x} LR: 0x{:x}      \n"
				"SP: 0x{:x} EA: 0x{:x}      \n"
				"CSR: {}        DSR: {}         \n"
				"Inited: {}     Run: {}         \n"
				"PSW: {}\n"
				//"Cycles: {}                 \n"
				""
				, view_str,
				core->PC, core->LR,
				core->SP, core->EA,
				core->CSR, core->DSR,
				core->inited, core->RunType,
				toBinaryString(core->PSW)
				//,core->cyclecount
			);
		auto hdc = GetDC(hWnd);
		SelectObject(hdc, textFont);
		RECT r{};
		GetClientRect(hWnd, &r);
		DrawTextA(hdc, final_view.data(), final_view.size(), &r, 0);
		RenderInput(hdc);
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
		case 5:
		{
			OPENFILENAMEA ofn;
			char fileName[MAX_PATH] = "";

			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = hWnd; // Window handle of the parent window
			ofn.lpstrFilter = "ROM Files (*.bin)\0*.bin\0All Files (*.*)\0*.*\0";
			ofn.lpstrFile = fileName;
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

			if (GetOpenFileNameA(&ofn))
				LoadRom(fileName);
			break;
		}
		case 6: {
			auto core = GetCSimU8Core();
			core->enablelog = !core->enablelog;
			break;
		}
		case 7: {
			auto core = GetCSimU8Core();
			auto dataseg = core->GetDataSegment0();
			memset(dataseg->Contents, 0, dataseg->Size);
			simu8api._SimReset();
			break;
		}
		case 8: {
			//char buffer[200];
			auto core = GetCSimU8Core();
			auto dataseg = core->GetDataSegment0();
			auto inputarea = GetInputArea(dataseg);
			PrintBuffer(inputarea, 200);
			//auto undobuf = (char*)inputarea + 200;
			//memcpy(buffer, inputarea, 200);
			//memcpy(inputarea, undobuf, 200);
			//memcpy(undobuf, buffer, 200);
			break;
		}
		case 9: {
			static bool toggled = false;
			auto diagki = (byte)219;
			if (toggled)
			{
				diagki = 0;
			}
			auto core = GetCSimU8Core();
			auto dataseg = core->GetDataSegment0();
			dataseg->Contents[0xf040] = diagki;
			//simu8api._WriteDataMemory(0xf040, 1, &diagki);
			if (!toggled)
			{
				simu8api._SimReset();
			}
			toggled = !toggled;
			break;
		}
		case 10: {
			auto core = GetCSimU8Core();
			char* romfile = new char[0x80000];
			memcpy(romfile, core->segments[0].Contents, 0x10000);
			memcpy(romfile + 0x10000, core->segments[2].Contents, 0x70000);
			OPENFILENAMEA ofn;
			char filename[MAX_PATH] = "";
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = NULL;
			ofn.lpstrFilter = "CWII ROM (*.bin)\0*.bin\0";
			ofn.lpstrFile = filename;
			ofn.nMaxFile = MAX_PATH;
			ofn.lpstrTitle = "Save ROM File";
			ofn.Flags = OFN_OVERWRITEPROMPT;

			if (GetSaveFileNameA(&ofn) == TRUE)
			{
				std::ofstream file(filename, std::ios::out | std::ios::binary);
				file.write(romfile, 0x80000);
				file.close();
			}
			delete romfile;
			break;
		}
		case 11: {
			auto core = GetCSimU8Core();
			char* romfile = new char[0x10000];
			memcpy(romfile, core->segments[1].Contents, 0x10000);
			OPENFILENAMEA ofn;
			char filename[MAX_PATH] = "";
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = NULL;
			ofn.lpstrFilter = "Data (*.bin)\0*.bin\0";
			ofn.lpstrFile = filename;
			ofn.nMaxFile = MAX_PATH;
			ofn.lpstrTitle = "Save Data File";
			ofn.Flags = OFN_OVERWRITEPROMPT;

			if (GetSaveFileNameA(&ofn) == TRUE)
			{
				std::ofstream file(filename, std::ios::out | std::ios::binary);
				file.write(romfile, 0x10000);
				file.close();
			}
			delete romfile;
			break;
		}
		case 12:
		{
			auto core = GetCSimU8Core();
			OPENFILENAMEA ofn;
			char fileName[MAX_PATH] = "";

			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = hWnd; // Window handle of the parent window
			ofn.lpstrFilter = "ROM Files (*.bin)\0*.bin\0All Files (*.*)\0*.*\0";
			ofn.lpstrFile = fileName;
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

			if (GetOpenFileNameA(&ofn))
			{
				auto data = readFileBytes(fileName);
				memcpy(core->segments[1].Contents, data.data(), 0x10000);
			}
			break;
		}
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
void Edit(const wchar_t* text, int x, int y, int cx, int cy, HWND win, int id = 0) {
	auto btn = CreateWindow(L"EDIT", text, WS_CHILD | WS_VISIBLE | ES_MULTILINE,
		x, y, cx, cy, win, (HMENU)id, 0, 0);
	SendMessage(btn, WM_SETFONT, (WPARAM)textFont, TRUE);
}

DWORD __stdcall load(LPVOID p) {
	auto hinst = GetModuleHandleW(0);
	MyRegisterClass(hinst);
	auto win = CreateWindowExW(WS_EX_TOOLWINDOW,
		L"CASIOHACK", L"CWII Control Panel",
		WS_OVERLAPPEDWINDOW & ~WS_SYSMENU & ~WS_SIZEBOX, 0, 0, 800, 600, 0, 0, hinst, 0);
	LoadFont();
	Btn(L"复位68", 680, 0, 100, 30, win, 1);
	Btn(L"复制", 680, 40 * 1, 100, 30, win, 2);
	Btn(L"粘贴", 680, 40 * 2, 100, 30, win, 3);
	Btn(L"清除历史记录", 680, 40 * 3, 100, 30, win, 4);
	Btn(L"更换 ROM", 680, 40 * 4, 100, 30, win, 5);
	Btn(L"切换日志开关", 680, 40 * 5, 100, 30, win, 6);
	Btn(L"完全重置", 680, 40 * 6, 100, 30, win, 7);
	Btn(L"DumpIntoCon", 680, 40 * 7, 100, 30, win, 8);
	Btn(L"切换Shift+7", 680, 40 * 8, 100, 30, win, 9);
	Btn(L"保存 ROM", 680, 40 * 9, 100, 30, win, 10);
	Btn(L"转储 数据", 680, 40 * 10, 100, 30, win, 11);
	Btn(L"加载 数据", 680, 40 * 11, 100, 30, win, 12);
	//Edit(L"", 0, 40 * 6, 680, 600, win, 10);
	SetTimer(win, 114514, 25, (TIMERPROC)NULL);
	ShowWindow(win, SW_SHOW);
	loadCasioFont();
	if (p == (LPVOID)1)
	{
		MSG m;
		while (GetMessage(&m, 0, 0, 0)) {
			TranslateMessage(&m);
			DispatchMessageW(&m);
		}
	}
	return 0;
}

int __stdcall WriteDataMemory_Dec(int iptr, int size, byte* ptr) {
	if (iptr == 61520 && size == 1)
	{
		if (std::filesystem::exists("rom.bin")) {
			std::cout << "Try to Load \"rom.bin\" into emulator.\n";
			LoadRom("rom.bin");
		}
		else {
			std::cout << "\"rom.bin\" not exist.You might place it into working directory to load custom roms.\n";
		}
	}
	if (iptr == 0x00088e01)
	{
		if (*ptr != 0)
			std::cout << "KI: " << std::hex << log2((int)*ptr) << " ";
	}
	if (iptr == 0x00088e02) {
		//GetCSimU8Core()->GetDataSegment0()->Contents[0xf040] = holdS7 ? 219 : 0;
		if (*ptr != 0)
			std::cout << "KO: " << std::hex << log2((int)*ptr) << "\n";
	}
	return simu8api._WriteDataMemory(iptr, size, ptr);
}
BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		std::cout << "CWIIEmuEnchant initialized.\n";
		std::cout << "Input memory: 0x88e01.\n";
		MH_Initialize();
		auto wdm = simu8api._WriteDataMemory;
		MH_CreateHook(wdm, WriteDataMemory_Dec, (LPVOID*)&simu8api._WriteDataMemory);
		MH_EnableHook(wdm);
		DisableThreadLibraryCalls(hModule);
		CreateThread(0, 0, load, (void*)1, 0, 0);
	}
	break;
	case DLL_PROCESS_DETACH:
		MH_Uninitialize();
		break;
	}
	return TRUE;
}

