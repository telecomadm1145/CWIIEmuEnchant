#pragma once
void* GetInputArea(CSegment* seg0) {
	return seg0->Contents + 0x9268;
}
template <typename T>
std::string to_hex(T value, size_t width = sizeof(T) * 2) {
	constexpr char hex_digits[] = "0123456789ABCDEF";
	std::string result;

	for (size_t i = 0; i < width; ++i) {
		result = hex_digits[value & 0xF] + result;
		value >>= 4;
	}

	return result;
}
void Reset68() {
	auto core = GetCSimU8Core();
	auto seg0 = core->GetDataSegment0();
	memset(seg0->Contents + 0x9068, 0, 0x200);
	simu8api._SimReset();
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
	auto res3 = SetClipboardData(CF_TEXT, global);
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
std::vector<char> readFileBytes(const std::string& fileName) {
	std::ifstream file(fileName, std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		std::cerr << "Failed to open file: " << fileName << std::endl;
		return std::vector<char>();
	}

	std::streamsize fileSize = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> buffer(fileSize);
	if (!file.read(buffer.data(), fileSize)) {
		std::cerr << "Failed to read file: " << fileName << std::endl;
		return std::vector<char>();
	}

	return buffer;
}

std::string GetRomName()
{
	auto core = GetCSimU8Core();
	auto ptr = &core->segments[2].Contents[0x61fee];
	return { ptr, ptr + 7 };
}

void LoadRom(const char* fileName) {
	auto core = GetCSimU8Core();
	auto bytes = readFileBytes(fileName);
	auto ptr = bytes.data();
	auto remaining = bytes.size();
	std::cout << "Rom file name: " << fileName << "\n" << "Rom file size: 0x" << std::hex << remaining << "\n";
	if (remaining != 0x80000) {
		std::cout << "Warning: your might loaded a wrong rom file.Correct rom file size: 0x80000.\n";
	}
	// 将每个段的数据加载进去
	for (size_t i = 0; i < 3; i++)
	{
		if (i == 1)
			continue;
		auto& seg = core->segments[i];
		memcpy(seg.Contents, ptr, min(seg.Size, remaining));
		ptr += seg.Size;
		remaining -= seg.Size;
	}
	std::cout << "Rom name: " << GetRomName() << "\n";
	simu8api._SimReset();
}