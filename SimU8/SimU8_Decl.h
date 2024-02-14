#pragma once
extern  "C" {
	int __stdcall WriteDataMemory(int iptr, int size, byte* ptr);
	int __stdcall WriteCodeMemory(int iptr, int size, byte* ptr);
	int __stdcall LogStart2(const char* unk);
	int __stdcall LogStart();
	int __stdcall LogStop();
	int __stdcall SimReset();
	int __stdcall SimStart();
	int __stdcall SimStop();
}
#define DeclApi1(x) _##x = (decltype(##x##)*)(GetProcAddress(simu8_dll,#x))
#define DeclApi2(x) decltype(##x##)* _##x##;
class SimU8Api {
public:
	SimU8Api() {
		simu8_dll = GetModuleHandleW(L"Real_SimU8.dll");
		DeclApi1(WriteDataMemory);
		DeclApi1(WriteCodeMemory);
		DeclApi1(LogStart2);
		DeclApi1(LogStart);
		DeclApi1(LogStop);
		DeclApi1(SimReset);
		DeclApi1(SimStart);
		DeclApi1(SimStop);
	}
	DeclApi2(WriteDataMemory);
	DeclApi2(WriteCodeMemory);
	DeclApi2(LogStart2);
	DeclApi2(LogStart);
	DeclApi2(LogStop);
	DeclApi2(SimReset);
	DeclApi2(SimStart);
	DeclApi2(SimStop);
private:
	HMODULE simu8_dll;
};
SimU8Api simu8api;