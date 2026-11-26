// Tests for DX9GFPlatform (time, UTF conversion). Plain asserts, no framework.
#include "DX9GFPlatform.h"

#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

static int g_failures = 0;
#define CHECK(cond) \
	do { if (!(cond)) { std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); ++g_failures; } } while (0)

using DX9GF::Platform::Utf8ToWide;
using DX9GF::Platform::WideToUtf8;

// Builds a wide string holding the single code point cp, in the platform's encoding.
static std::wstring WideFromCodePoint(char32_t cp)
{
	std::wstring w;
	if (sizeof(wchar_t) == 2 && cp >= 0x10000) {
		cp -= 0x10000;
		w += static_cast<wchar_t>(0xD800 + (cp >> 10));
		w += static_cast<wchar_t>(0xDC00 + (cp & 0x3FF));
	}
	else {
		w += static_cast<wchar_t>(cp);
	}
	return w;
}

static void TestTicks()
{
	const auto t1 = DX9GF::Platform::GetTicksMs();
	std::this_thread::sleep_for(std::chrono::milliseconds(30));
	const auto t2 = DX9GF::Platform::GetTicksMs();
	CHECK(t2 >= t1);
	CHECK(t2 - t1 >= 25);   // slept 30 ms; allow scheduler slack
	CHECK(t2 - t1 < 2000);
	CHECK(t1 < 60ull * 1000); // measured from first call, not from boot
}

static void TestKnownVectors()
{
	CHECK(Utf8ToWide("").empty());
	CHECK(WideToUtf8(L"").empty());
	CHECK(Utf8ToWide("hello") == L"hello");
	CHECK(WideToUtf8(L"hello") == "hello");

	// Vietnamese: the game's primary non-ASCII text.
	const std::string viUtf8 = "\xC4\x90\xC3\xA2y l\xC3\xA0 \xE1\xBA\xA3nh";
	const std::wstring viWide = L"\u0110\u00E2y l\u00E0 \u1EA3nh";
	CHECK(Utf8ToWide(viUtf8) == viWide);
	CHECK(WideToUtf8(viWide) == viUtf8);

	// Supplementary plane (surrogate pair on Windows, single unit elsewhere).
	CHECK(Utf8ToWide("\xF0\x9F\x98\x80") == WideFromCodePoint(0x1F600));
	CHECK(WideToUtf8(WideFromCodePoint(0x1F600)) == "\xF0\x9F\x98\x80");

	// Embedded NUL is data, not a terminator.
	const std::string withNul("a\0b", 3);
	CHECK(Utf8ToWide(withNul).size() == 3);
	CHECK(WideToUtf8(Utf8ToWide(withNul)) == withNul);
}

static void TestInvalidInput()
{
	const std::wstring fffd = L"\uFFFD";
	CHECK(Utf8ToWide("\xFF") == fffd);                        // never valid
	CHECK(Utf8ToWide("\x80") == fffd);                        // stray continuation
	CHECK(Utf8ToWide("\xE2\x82") == fffd);                    // truncated 3-byte sequence: one replacement
	CHECK(Utf8ToWide("\xE2\x82z") == fffd + L"z");            // truncated, then valid data resumes
	CHECK(Utf8ToWide("\xC0\x80") == fffd + fffd);             // overlong
	CHECK(Utf8ToWide("\xED\xA0\x80") == fffd + fffd + fffd);  // UTF-8-encoded surrogate
	CHECK(Utf8ToWide("\xF4\x90\x80\x80") == fffd + fffd + fffd + fffd); // > U+10FFFF

	// Lone surrogates in a UTF-16 wstring must not leak into the UTF-8 output.
	if (sizeof(wchar_t) == 2) {
		CHECK(WideToUtf8(std::wstring(1, static_cast<wchar_t>(0xD800))) == "\xEF\xBF\xBD");
		CHECK(WideToUtf8(std::wstring(1, static_cast<wchar_t>(0xDC00))) == "\xEF\xBF\xBD");
		std::wstring highThenAscii; highThenAscii += static_cast<wchar_t>(0xD800); highThenAscii += L'a';
		CHECK(WideToUtf8(highThenAscii) == "\xEF\xBF\xBD" "a");
	}
}

static void TestRoundTripAllCodePoints()
{
	size_t checked = 0;
	for (char32_t cp = 0; cp <= 0x10FFFF; ++cp) {
		if (cp >= 0xD800 && cp <= 0xDFFF) continue;
		const std::wstring w = WideFromCodePoint(cp);
		const std::string u8 = WideToUtf8(w);
		if (Utf8ToWide(u8) != w) { CHECK(!"round trip"); std::fprintf(stderr, "  cp=U+%X\n", (unsigned)cp); break; }
		++checked;
	}
	CHECK(checked == 0x110000 - 0x800);
}

#if defined(_WIN32)
// Cross-check against the Win32 converters the code used before this layer existed.
static std::wstring Win32Utf8ToWide(const std::string& s)
{
	if (s.empty()) return {};
	int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
	std::wstring w(n, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), w.data(), n);
	return w;
}
static std::string Win32WideToUtf8(const std::wstring& w)
{
	if (w.empty()) return {};
	int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
	std::string s(n, '\0');
	WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), s.data(), n, nullptr, nullptr);
	return s;
}

static std::wstring CollapseReplacementRuns(const std::wstring& s)
{
	std::wstring out;
	for (wchar_t c : s) {
		if (c == L'\uFFFD' && !out.empty() && out.back() == L'\uFFFD') continue;
		out += c;
	}
	return out;
}

static void TestMatchesWin32()
{
	// Valid input and the common malformed cases must match Win32 exactly.
	const std::vector<std::string> exact = {
		"", "abc", "\xC4\x90\xC3\xA2y", "\xF0\x9F\x98\x80", "\xFF", "\x80", "\xE2\x82", "\xE2\x82z",
		"\xC0\x80", "a\xC3", "\xF0\x9F\x98", "\xF0\x9F\x98z",
		"ok\xE1\xBA\xA3\xFF" "end", "\xC2\xC2\xA9",
	};
	for (const auto& in : exact) {
		const std::wstring mine = Utf8ToWide(in);
		const std::wstring theirs = Win32Utf8ToWide(in);
		if (mine != theirs) {
			std::fprintf(stderr, "Utf8ToWide differs from MultiByteToWideChar for input of %zu bytes:", in.size());
			for (unsigned char c : in) std::fprintf(stderr, " %02X", c);
			std::fprintf(stderr, "\n");
		}
		CHECK(mine == theirs);
	}
	// Overlong / out-of-range sequences: Win32 emits fewer U+FFFD than the maximal-subpart
	// rule. Require the same result once runs of replacement characters are collapsed.
	const std::vector<std::string> relaxed = {
		"\xE0\x80\x80", "\xF0\x80\x80\x80", "\xF4\x90\x80\x80", "\xED\xA0\x80", "x\xE0\x80\x80y", "x\xF4\x90\x80\x80y",
	};
	for (const auto& in : relaxed) {
		CHECK(CollapseReplacementRuns(Utf8ToWide(in)) == CollapseReplacementRuns(Win32Utf8ToWide(in)));
	}
	const std::vector<std::wstring> wides = {
		L"", L"abc", L"\u0110\u00E2y", WideFromCodePoint(0x1F600),
		std::wstring(1, static_cast<wchar_t>(0xD800)), std::wstring(1, static_cast<wchar_t>(0xDC00)),
		std::wstring(L"x") + static_cast<wchar_t>(0xD800) + L"y",
		std::wstring(1, static_cast<wchar_t>(0xDC00)) + static_cast<wchar_t>(0xD800),
	};
	for (const auto& in : wides) {
		const std::string mine = WideToUtf8(in);
		const std::string theirs = Win32WideToUtf8(in);
		if (mine != theirs) std::fprintf(stderr, "WideToUtf8 differs from WideCharToMultiByte (len %zu)\n", in.size());
		CHECK(mine == theirs);
	}
}
#endif

int main()
{
	TestTicks();
	TestKnownVectors();
	TestInvalidInput();
	TestRoundTripAllCodePoints();
#if defined(_WIN32)
	TestMatchesWin32();
#endif
	if (g_failures == 0) std::puts("platform_test: all tests passed");
	else std::fprintf(stderr, "platform_test: %d check(s) failed\n", g_failures);
	return g_failures == 0 ? 0 : 1;
}
