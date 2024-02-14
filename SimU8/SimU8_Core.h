#pragma once
#include <Windows.h>
class CSegment {
public:
	char Unk[4];
	byte* Contents;
	size_t Size;
	char Unk2[8];
	int StartAddress;
	int EndAddress;
};
class CSimU8Core {
public:
	void* vtable;
	char unk[40];
	CSegment segments[3];
	int unk3;
	byte enableBCD;
	char unk2[55];
	byte registers[16];
	USHORT PC;
	USHORT LR;
	USHORT ELR1;
	USHORT ELR2;
	USHORT ELR3;
	byte CSR;
	byte LCSR;
	byte ECSR[3];
	byte PSW;
	byte EPSW[3];
	USHORT SP;
	USHORT EA;
	byte DSR;
	byte DSRPrefix;
	char unk4[11];
	byte RunType;
	char unk5[2];
	byte inited;
	int cyclecount;
	int enablelog;
	int startcode;
	int endcode;
	int startdata;
	int enddata;
	int startrom;
	int endrom;
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