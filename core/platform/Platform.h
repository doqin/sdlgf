#pragma once
// Platform layer for the code that is not tied to a particular graphics/input/audio
// API: Win32 type stand-ins, time, debug output, message boxes and UTF conversion.
//
// On Windows this simply pulls in <windows.h> (the types the code base was written
// against). Everywhere else it provides the handful of Win32 types the game code
// still uses, so non-Windows builds don't need <windows.h>. Later phases of the SDL3
// port shrink this list further as the code that uses those types goes away.
#include <cstdint>
#include <string>
#include <string_view>

#if defined(_WIN32) && !defined(SDLGF_PLATFORM_FORCE_PORTABLE_TYPES)
#	ifndef NOMINMAX
#		define NOMINMAX // keep std::min/std::max usable; the code base never relied on the macros
#	endif
#	include <windows.h>
#else
// Sizes match the Win32 definitions (LONG/DWORD are 32-bit on every platform; 'long'
// is not on LP64 systems), so structs like RECT keep their layout.
using BYTE = std::uint8_t;
using WORD = std::uint16_t;
using DWORD = std::uint32_t;
using UINT = std::uint32_t;
using UINT32 = std::uint32_t;
using INT = std::int32_t;
using LONG = std::int32_t;
using ULONG = std::uint32_t;
using BOOL = std::int32_t;
using WCHAR = wchar_t;
using HRESULT = std::int32_t;

#	ifndef TRUE
#		define TRUE 1
#	endif
#	ifndef FALSE
#		define FALSE 0
#	endif
#	ifndef MAX_PATH
#		define MAX_PATH 260
#	endif
#	ifndef S_OK
#		define S_OK (static_cast<HRESULT>(0))
#	endif
#	ifndef E_FAIL
#		define E_FAIL (static_cast<HRESULT>(0x80004005))
#	endif
#	ifndef SUCCEEDED
#		define SUCCEEDED(hr) (static_cast<HRESULT>(hr) >= 0)
#	endif
#	ifndef FAILED
#		define FAILED(hr) (static_cast<HRESULT>(hr) < 0)
#	endif

struct RECT {
	LONG left;
	LONG top;
	LONG right;
	LONG bottom;
};

struct POINT {
	LONG x;
	LONG y;
};

struct SIZE {
	LONG cx;
	LONG cy;
};
#endif

namespace sdlgf::Platform {
	/// <summary>
	/// Milliseconds on a monotonic clock, measured from the first call. Replaces
	/// GetTickCount64(); starting near zero keeps float conversions precise for
	/// animation phases (sin(ticks * k)).
	/// </summary>
	unsigned long long GetTicksMs();

	/// <summary>
	/// Writes a line of diagnostic output (debugger output window on Windows,
	/// stderr elsewhere). A trailing newline is the caller's responsibility.
	/// </summary>
	void DebugLog(const std::string& message);

	/// <summary>
	/// Shows a blocking error dialog where the platform has one; always also logs to stderr.
	/// </summary>
	void ShowErrorMessage(const std::string& title, const std::string& message);

	/// <summary>
	/// Absolute path of the running executable, UTF-8. Empty if it cannot be determined.
	/// </summary>
	std::string GetExecutablePath();

	/// <summary>
	/// Directory containing the running executable, UTF-8, without a trailing separator.
	/// </summary>
	std::string GetExecutableDirectory();

	/// <summary>
	/// Converts UTF-8 to the platform's wide string (UTF-16 on Windows, UTF-32 elsewhere).
	/// Invalid sequences become U+FFFD. Valid input converts identically to
	/// MultiByteToWideChar; for malformed input (esp. overlong / out-of-range sequences)
	/// the NUMBER of U+FFFD emitted can differ from Win32 (this follows the Unicode
	/// "maximal subpart" rule), but valid text around the damage is preserved.
	/// </summary>
	std::wstring Utf8ToWide(std::string_view utf8);

	/// <summary>
	/// Converts a platform wide string to UTF-8. Unpaired surrogates / invalid code
	/// points become U+FFFD.
	/// </summary>
	std::string WideToUtf8(std::wstring_view wide);

	/// <summary>
	/// Decodes a platform wide string (UTF-16 or UTF-32) to Unicode code points. Unpaired
	/// surrogates / invalid values become U+FFFD.
	/// </summary>
	std::u32string WideToUtf32(std::wstring_view wide);

	/// <summary>
	/// Encodes Unicode code points as UTF-8. Invalid values become U+FFFD.
	/// </summary>
	std::string Utf32ToUtf8(std::u32string_view codePoints);
}
