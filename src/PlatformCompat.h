#pragma once

// Small cross-platform compatibility helpers.
//
// On Windows these types/macros are provided implicitly by <windows.h>.
// On macOS we provide the equivalents so the shared source compiles unchanged.

#ifdef __APPLE__
#include <cstdint>
#include <cstddef>
#include <algorithm>

#ifndef byte
typedef unsigned char byte;
#endif

// `HMENU` is a Win32 type used as a member of PluginGUI. The SDK's Apple shim
// defines it as a macro, so only provide a typedef when it is not already set.
#ifndef HMENU
typedef intptr_t HMENU;
#endif

#ifndef min
using std::min;
#endif

#ifndef max
using std::max;
#endif

// Convert a wide string to UTF-8 bytes. The macOS host expects UTF-8 while the
// Windows source historically used wcstombs (ANSI). The provided size is treated
// as a maximum number of bytes (including the terminating NUL).
inline std::size_t platformWideToBytes(char* dst, const wchar_t* src, std::size_t maxBytes)
{
	if (maxBytes == 0)
		return 0;

	std::size_t written = 0;
	for (const wchar_t* p = src; *p != 0; ++p)
	{
		unsigned int cp = (unsigned int)*p;
		unsigned char buf[4];
		int len;
		if (cp < 0x80)
		{
			buf[0] = (unsigned char)cp;
			len = 1;
		}
		else if (cp < 0x800)
		{
			buf[0] = (unsigned char)(0xC0 | (cp >> 6));
			buf[1] = (unsigned char)(0x80 | (cp & 0x3F));
			len = 2;
		}
		else if (cp < 0x10000)
		{
			buf[0] = (unsigned char)(0xE0 | (cp >> 12));
			buf[1] = (unsigned char)(0x80 | ((cp >> 6) & 0x3F));
			buf[2] = (unsigned char)(0x80 | (cp & 0x3F));
			len = 3;
		}
		else
		{
			buf[0] = (unsigned char)(0xF0 | (cp >> 18));
			buf[1] = (unsigned char)(0x80 | ((cp >> 12) & 0x3F));
			buf[2] = (unsigned char)(0x80 | ((cp >> 6) & 0x3F));
			buf[3] = (unsigned char)(0x80 | (cp & 0x3F));
			len = 4;
		}

		if (written + (std::size_t)len >= maxBytes)
			break;

		for (int i = 0; i < len; ++i)
			dst[written++] = (char)buf[i];
	}

	dst[written] = '\0';
	return written;
}

#else
#include <cstdlib>
#define platformWideToBytes wcstombs
#endif
