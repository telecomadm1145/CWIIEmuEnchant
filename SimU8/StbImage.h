#pragma once
#include <utility>
#include <memory>
extern "C" {
	unsigned char* stbi_load(char const* filename, int* x, int* y, int* comp, int req_comp);
}
class Image
{
private:
	unsigned char* scan0 = nullptr;
	int w = 0;
	int h = 0;
	int c = 0;

public:
	// Default constructor
	Image() = default;

	// Constructor with specified width, height, and channels
	Image(int w, int h, int channels = 3)
		: w(w), h(h), c(channels), scan0(new unsigned char[w * h * channels])
	{}

	// Constructor with specified pointer, width, height, and channels
	Image(void* scan0, int w, int h, int channels = 3)
		: w(w), h(h), c(channels), scan0((unsigned char*)scan0)
	{}

	// Constructor with specified filename and channels
	Image(const char* filename, int channels = 3)
	{
		int _;
		scan0 = stbi_load(filename, &w, &h, &_, channels);
		c = _;
	}

	// Copy constructor
	Image(const Image& other)
		: w(other.w), h(other.h), c(other.c)
	{
		scan0 = new unsigned char[w * h * c];
		std::memcpy(scan0, other.scan0, w * h * c);
	}

	// Move constructor
	Image(Image&& other) noexcept
		: w(other.w), h(other.h), c(other.c), scan0(other.scan0)
	{
		other.scan0 = nullptr;
	}

	// Copy assignment operator
	Image& operator=(Image& rhs)
	{
		if (this != &rhs) {
			if (scan0 != 0)
				delete[] scan0;
			w = rhs.w;
			h = rhs.h;
			c = rhs.c;
			scan0 = rhs.scan0;
			rhs.scan0 = nullptr;
		}
		return *this;
	}

	// Move assignment operator
	Image& operator=(Image&& rhs) noexcept
	{
		if (this != &rhs) {
			if (scan0 != 0)
				delete[] scan0;
			w = rhs.w;
			h = rhs.h;
			c = rhs.c;
			scan0 = rhs.scan0;
			rhs.scan0 = nullptr;
		}
		return *this;
	}

	unsigned char* Scan0()
	{
		return scan0;
	}
	int Width()
	{
		return w;
	}
	int Height()
	{
		return h;
	}
	int Channels()
	{
		return c;
	}

	// Destructor
	~Image()
	{
		if (scan0 != 0)
			delete[] scan0;
	}
#if _WINDOWS == 1
	HBITMAP GenerateDIBitmap()
	{
		BITMAPINFO bmi;
		ZeroMemory(&bmi, sizeof(BITMAPINFO));
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = w;
		bmi.bmiHeader.biHeight = -h; // Negative height indicates a top-down DIB
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = c * 8;
		bmi.bmiHeader.biCompression = BI_RGB;

		HDC hdc = GetDC(NULL);
		HBITMAP hBitmap = CreateDIBitmap(hdc, &bmi.bmiHeader, CBM_INIT, scan0, &bmi, DIB_RGB_COLORS);
		ReleaseDC(NULL, hdc);

		return hBitmap;
	}
#endif
};