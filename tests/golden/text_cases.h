#pragma once
// The text scenes behind the golden images, shared by the golden generator (which rendered them with
// the old D3DXFont renderer) and the tests (which check the SDL text module against those images).
// Changing anything here invalidates tests/golden/text_*.png and the metrics files.
#include <vector>

namespace golden {

	/// One row of a text sheet (text_<font><size>.png). Rows are `RowStep(fontSize)` apart,
	/// starting at y = 8, with the text's top-left at (x, y + yFraction).
	struct TextRow {
		const char* id;
		const wchar_t* text;
		unsigned color = 0xFFFFFFFF;   // 0xAARRGGBB
		bool bold = false;
		bool outline = false;
		float outlineThickness = 1.0f;
		float maxWidth = 0.0f;
		float zoom = 1.0f;             // camera zoom (the game scales text this way)
		float x = 8.0f;
		float yFraction = 0.0f;
	};

	inline float RowStep(int fontSize) { return static_cast<float>(fontSize) * 4.5f; }

	inline const std::vector<TextRow>& TextRows()
	{
		static const std::vector<TextRow> rows = {
			{ "latin",       L"The quick brown fox jumps over 0123456789" },
			{ "vietnamese",  L"T\u00F4i, sinh vi\u00EAn UIT n\u0103m 6 b\u1ECB h\u00FAt v\u00E0o cyberspace" },
			{ "diacritics",  L"\u1EAE \u1EB0 \u1EB2 \u1EB4 \u1EB6 \u0110 \u0111 \u01A0 \u01A1 \u01AF \u01B0 \u1EC5 \u1EE3" },
			{ "punctuation", L"!@#$%^*()-_=+[]{};:'\",.<>/?\\|~`" },
			{ "ampersand",   L"UI & Icons: A && B & C" },
			{ "amp_ab",      L"AB" },
			{ "amp_a_b",     L"A&B" },
			{ "amp_a_ampamp_b", L"A&&B" },
			{ "multiline",   L"line one\nline two\r\nline three" },
			{ "wrapped",     L"A long sentence that has to wrap onto several lines because it exceeds the maximum width.", 0xFFFFFFFF, false, false, 1.0f, 300.0f },
			{ "color",       L"Red text", 0xFFFF0000 },
			{ "alpha",       L"Half transparent", 0x80FFFFFF },
			{ "bold",        L"Bold text", 0xFFFFFFFF, true },
			{ "outline1",    L"Outline 1", 0xFFFFFFFF, false, true, 1.0f },
			{ "outline2",    L"Outline 2 bold", 0xFFFFD700, true, true, 2.0f },
			{ "zoom2",       L"Zoom 2x", 0xFFFFFFFF, false, false, 1.0f, 0.0f, 2.0f },
			{ "zoom1_5",     L"Zoom 1.5x", 0xFFFFFFFF, false, false, 1.0f, 0.0f, 1.5f },
			{ "fractional",  L"Fractional position", 0xFFFFFFFF, false, false, 1.0f, 0.0f, 1.0f, 8.5f, 0.5f },
		};
		return rows;
	}

	/// Layout edge cases (text_probe.png, probe_metrics.txt), font 135OpenPixel v2.3 at 13 px.
	struct Probe {
		const char* id;
		const wchar_t* text;
		float maxWidth;
	};

	inline const std::vector<Probe>& Probes()
	{
		static const std::vector<Probe> probes = {
			{ "empty",           L"",                     0 },
			{ "single_space",    L" ",                    0 },
			{ "only_newline",    L"\n",                   0 },
			{ "trailing_nl",     L"a\n",                  0 },
			{ "leading_nl",      L"\na",                  0 },
			{ "blank_line",      L"a\n\nb",               0 },
			{ "double_space",    L"a  b",                 0 },
			{ "leading_spaces",  L"  lead",               0 },
			{ "trailing_spaces", L"trail  ",              0 },
			{ "tab",             L"a\tb",                 0 },
			{ "cr_only",         L"a\rb",                 0 },
			{ "crlf",            L"a\r\nb",               0 },
			{ "long_word",       L"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", 100 },
			{ "wrap_words",      L"word word word word word", 60 },
			{ "wrap_exact_fit",  L"AAAA BBBB",            54 },
			{ "wrap_one_short",  L"AAAA BBBB",            53 },
			{ "wrap_tiny",       L"abc",                  3 },
			{ "wrap_newline",    L"one two\nthree four five", 50 },
			{ "wrap_lead_space", L"aaaaaaaa  bbbb",       50 },
			{ "wrap_trail_space",L"aaaaaaaa bbbb ",       50 },
			{ "wrap_long_after", L"ab AAAAAAAAAAAAAAAAAAAAAA cd", 60 },
			{ "wrap_vietnamese", L"T\u00F4i, sinh vi\u00EAn UIT n\u0103m 6 b\u1ECB h\u00FAt v\u00E0o cyberspace", 120 },
		};
		return probes;
	}

	/// The fonts the sheets were rendered with: family, cell height, file tag (text_<tag>.png), TTF file.
	struct FontCase {
		const wchar_t* family;
		int size;
		const char* tag;
		const char* file;
	};

	inline const std::vector<FontCase>& Fonts()
	{
		static const std::vector<FontCase> fonts = {
			{ L"135OpenPixel v2.3",     13, "openpixel13",      "135openpixel-v2-3.ttf" },
			{ L"135OpenPixel v2.3",     26, "openpixel26",      "135openpixel-v2-3.ttf" },
			{ L"135OpenPixel v2.3",     39, "openpixel39",      "135openpixel-v2-3.ttf" },
			{ L"CardPixel",             18, "cardpixel18",      "cardpixel.ttf" },
			{ L"CardPixelTop",          18, "cardpixeltop18",   "cardpixel-top.ttf" },
			{ L"CardPixelBottom",       18, "cardpixelbottom18","cardpixel-bottom.ttf" },
			{ L"Arcade Among 2 R46PV",  24, "arcade24",         "arcade-among-2-r46pv.ttf" },
		};
		return fonts;
	}
}
