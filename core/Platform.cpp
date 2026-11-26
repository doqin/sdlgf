#include "Platform.h"
#include <chrono>
#include <SDL3/SDL_messagebox.h>
#include <cstdio>
#include <type_traits>

#if defined(__APPLE__)
#	include <mach-o/dyld.h>
#elif !defined(_WIN32)
#	include <limits.h>
#	include <unistd.h>
#endif

unsigned long long sdlgf::Platform::GetTicksMs()
{
	using Clock = std::chrono::steady_clock;
	static const Clock::time_point origin = Clock::now();
	return static_cast<unsigned long long>(
		std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - origin).count());
}

void sdlgf::Platform::DebugLog(const std::string& message)
{
#if defined(_WIN32)
	OutputDebugStringA(message.c_str());
#else
	std::fputs(message.c_str(), stderr);
#endif
}

void sdlgf::Platform::ShowErrorMessage(const std::string& title, const std::string& message)
{
	std::fprintf(stderr, "[%s] %s\n", title.c_str(), message.c_str());
	// Safe to call before SDL_Init, which matters for startup failures.
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title.c_str(), message.c_str(), nullptr);
}

std::string sdlgf::Platform::GetExecutablePath()
{
#if defined(_WIN32)
	std::wstring buffer(MAX_PATH, L'\0');
	for (;;) {
		const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
		if (length == 0) return {};
		if (length < buffer.size()) {
			buffer.resize(length);
			return WideToUtf8(buffer);
		}
		buffer.resize(buffer.size() * 2); // path was truncated
	}
#elif defined(__APPLE__)
	uint32_t size = 0;
	_NSGetExecutablePath(nullptr, &size);
	std::string buffer(size, '\0');
	if (_NSGetExecutablePath(buffer.data(), &size) != 0) return {};
	buffer.resize(std::char_traits<char>::length(buffer.c_str()));
	return buffer;
#else
	std::string buffer(PATH_MAX, '\0');
	for (;;) {
		const ssize_t length = readlink("/proc/self/exe", buffer.data(), buffer.size());
		if (length < 0) return {};
		if (static_cast<size_t>(length) < buffer.size()) {
			buffer.resize(static_cast<size_t>(length));
			return buffer;
		}
		buffer.resize(buffer.size() * 2);
	}
#endif
}

std::string sdlgf::Platform::GetExecutableDirectory()
{
	const std::string path = GetExecutablePath();
	const size_t slash = path.find_last_of("/\\");
	if (slash == std::string::npos) return {};
	return path.substr(0, slash);
}

namespace {
	constexpr char32_t kReplacement = 0xFFFD;

	// Decodes one code point starting at s[i]. On malformed input yields U+FFFD and
	// consumes the maximal valid prefix (so one bad sequence gives one replacement
	// character, as in the Unicode "maximal subpart" practice used by Windows).
	char32_t DecodeUtf8(std::string_view s, size_t& i)
	{
		const auto byte = [&](size_t k) { return static_cast<unsigned char>(s[k]); };
		const unsigned char lead = byte(i);
		if (lead < 0x80) { ++i; return lead; }

		int extra;
		char32_t cp;
		unsigned char lo = 0x80, hi = 0xBF; // allowed range of the FIRST continuation byte
		if (lead >= 0xC2 && lead <= 0xDF) { extra = 1; cp = lead & 0x1F; }
		else if (lead >= 0xE0 && lead <= 0xEF) {
			extra = 2; cp = lead & 0x0F;
			if (lead == 0xE0) lo = 0xA0;      // no overlongs
			if (lead == 0xED) hi = 0x9F;      // no surrogates
		}
		else if (lead >= 0xF0 && lead <= 0xF4) {
			extra = 3; cp = lead & 0x07;
			if (lead == 0xF0) lo = 0x90;      // no overlongs
			if (lead == 0xF4) hi = 0x8F;      // <= U+10FFFF
		}
		else { ++i; return kReplacement; }    // stray continuation byte, C0/C1, F5..FF

		size_t k = i + 1;
		for (int n = 0; n < extra; ++n, ++k) {
			if (k >= s.size()) { i = k; return kReplacement; }
			const unsigned char c = byte(k);
			const unsigned char minC = (n == 0) ? lo : 0x80;
			const unsigned char maxC = (n == 0) ? hi : 0xBF;
			if (c < minC || c > maxC) { i = k; return kReplacement; } // truncated: resume at c
			cp = (cp << 6) | (c & 0x3F);
		}
		i = k;
		return cp;
	}

	void AppendUtf8(std::string& out, char32_t cp)
	{
		if (cp < 0x80) { out += static_cast<char>(cp); }
		else if (cp < 0x800) {
			out += static_cast<char>(0xC0 | (cp >> 6));
			out += static_cast<char>(0x80 | (cp & 0x3F));
		}
		else if (cp < 0x10000) {
			out += static_cast<char>(0xE0 | (cp >> 12));
			out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
			out += static_cast<char>(0x80 | (cp & 0x3F));
		}
		else {
			out += static_cast<char>(0xF0 | (cp >> 18));
			out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
			out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
			out += static_cast<char>(0x80 | (cp & 0x3F));
		}
	}

	void AppendWide(std::wstring& out, char32_t cp)
	{
		if constexpr (sizeof(wchar_t) == 2) {
			if (cp >= 0x10000) {
				cp -= 0x10000;
				out += static_cast<wchar_t>(0xD800 + (cp >> 10));
				out += static_cast<wchar_t>(0xDC00 + (cp & 0x3FF));
				return;
			}
		}
		out += static_cast<wchar_t>(cp);
	}

	// Decodes one code point from a wide string (UTF-16 or UTF-32 depending on wchar_t).
	char32_t DecodeWide(std::wstring_view s, size_t& i)
	{
		const char32_t unit = static_cast<char32_t>(static_cast<std::make_unsigned_t<wchar_t>>(s[i++]));
		if constexpr (sizeof(wchar_t) == 2) {
			if (unit >= 0xD800 && unit <= 0xDBFF) {
				if (i < s.size()) {
					const char32_t next = static_cast<char32_t>(static_cast<std::make_unsigned_t<wchar_t>>(s[i]));
					if (next >= 0xDC00 && next <= 0xDFFF) {
						++i;
						return 0x10000 + ((unit - 0xD800) << 10) + (next - 0xDC00);
					}
				}
				return kReplacement; // unpaired high surrogate
			}
			if (unit >= 0xDC00 && unit <= 0xDFFF) return kReplacement; // unpaired low surrogate
			return unit;
		}
		else {
			if ((unit >= 0xD800 && unit <= 0xDFFF) || unit > 0x10FFFF) return kReplacement;
			return unit;
		}
	}
}

std::wstring sdlgf::Platform::Utf8ToWide(std::string_view utf8)
{
	std::wstring out;
	out.reserve(utf8.size());
	for (size_t i = 0; i < utf8.size();) {
		AppendWide(out, DecodeUtf8(utf8, i));
	}
	return out;
}

std::string sdlgf::Platform::WideToUtf8(std::wstring_view wide)
{
	std::string out;
	out.reserve(wide.size());
	for (size_t i = 0; i < wide.size();) {
		AppendUtf8(out, DecodeWide(wide, i));
	}
	return out;
}
